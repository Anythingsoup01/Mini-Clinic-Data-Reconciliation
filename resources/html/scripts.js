function setupTags(inputId, containerId) {
  const input = document.getElementById(inputId);
  const container = document.getElementById(containerId);
  input.addEventListener('keydown', (e) => {
    if (e.key === 'Enter' && input.value.trim() !== "") {
      e.preventDefault();
      createTag(input.value.trim(), container, input);
      input.value = "";
    } else if (e.key === 'Backspace' && input.value === "") {
      // Find the last .tag element inside the container
      const tags = container.querySelectorAll('.tag');
      if (tags.length > 0) {
        tags[tags.length - 1].remove();
      }
    }
  });
}

function createTag(text, container, input) {
  const tag = document.createElement('span');
  tag.className = 'tag';
  tag.innerHTML = `${text} <span class="remove-btn" onclick="this.parentElement.remove()">x</span>`;
  container.insertBefore(tag, input);
}


setupTags("reconcile-condition-input", "reconcile-condition-tags");
setupTags("reconcile-lab-input", "reconcile-lab-tags");


setupTags("validate-condition-input", "validate-condition-tags");
setupTags("validate-allergy-input", "validate-allergy-tags");
setupTags("validate-med-input", "validate-med-tags");
setupTags("validate-vitals-input", "validate-vitals-tags");


function addReconcileRow() {
  const tbody = document.getElementById('reconcile-med-body');
  const newRow = document.createElement('tr');

  newRow.innerHTML = `
    <td><input type="text" placeholder="Source System" id="system-source"></td>
    <td><input type="text" placeholder="Medication & Dosage" id="medication-info"></td>
    <td><input type="text" placeholder="YYYY-MM-DD" id="update-date"></td>
    <td><select>
      <option value="">Select...</option>
      <option value="low">Low</option>
      <option value="medium">Medium</option>
      <option value="high">High</option>
    </select></td>
    <td><button type="button" onclick="this.parentElement.parentElement.remove()">Remove</button></td>
    `;

  tbody.appendChild(newRow);
}

function captureReconcileData() {

  const patientData = {
    patient_context: {
      age: document.getElementById("reconcile-patient-age").value,
      conditions: Array.from(document.querySelectorAll("#reconcile-condition-tags .tag")).map(t => t.textContent.replace('x', '').trim()),
      recent_labs: Array.from(document.querySelectorAll("#reconcile-lab-tags .tag")).map(t => t.textContent.replace('x', '').trim())
    },
    sources: []
  };

  const rows = document.querySelectorAll('#reconcile-med-body tr');

  rows.forEach(row => {
    const inputs = row.querySelectorAll('input');
    patientData.sources.push({
      system: inputs[0].value,
      medication: inputs[1].value,
      last_updated: inputs[2].value,
      source_reliability: row.source_reliability
    });
  });
  return patientData;
}

function displayReconciliationResults(data) {
  const tbody = document.getElementById('reconcile-body');
  const outputSection = document.getElementById('reconcile-output');

  // Clear and show
  tbody.innerHTML = '';
  outputSection.style.display = 'block';

  // Ensure we handle either a single object or an array
  const meds = Array.isArray(data) ? data : [data];

  meds.forEach(item => {
    const row = tbody.insertRow();
    const safetyStyle = item.clinical_safety_check === "Pass" 
      ? "color: #2e7d32; font-weight: bold;" 
      : "color: #c62828; font-weight: bold;";

    // Handle the actions list safely
    const actionsHtml = Array.isArray(item.recommended_actions) 
      ? `<ul style="margin: 0; padding-left: 1rem;">${item.recommended_actions.map(a => `<li>${a}</li>`).join('')}</ul>`
      : item.recommended_actions;

    row.innerHTML = `
      <td><strong>${item.reconciled_medication}</strong></td>
      <td>
        <progress value="${item.confidence_score}" max="100" style="margin:0;"></progress>
        <small>${item.confidence_score}%</small>
      </td>
      <td>${item.reasoning}</td>
      <td>${actionsHtml}</td>
          <td style="${safetyStyle}">${item.clinical_safety_check}</td>
      `;
  });
}

async function submitReconciliation() {
  const data = captureReconcileData();

  try {
    response = await fetch('/api/reconcile/medication', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(data)
    });

    if (!response.ok) {
      const errorData = await response.json();
      console.error("Server Error:", errorData);
      submitReconciliation();
    }

    const jsonData = await response.json();
    displayReconciliationResults(jsonData);
  }
  catch (error) {
    console.error("Fetch error: ", error);
  }
}

function displayValidationResults(data) {
  // Show the results section
  const outputSection = document.getElementById('validation-output');
  outputSection.style.display = 'block';

  // Update Overall Score
  document.getElementById('res-overall-score').innerText = data.overall_score;

  // Update Progress Bars & Numerical Labels
  const setProgress = (id, val) => {
    const el = document.getElementById(id);
    el.value = val;
  };

  setProgress('prog-completeness', data.breakdown.completeness);
  setProgress('prog-accuracy', data.breakdown.accuracy);
  setProgress('prog-timeliness', data.breakdown.timeliness);
  setProgress('prog-plausibility', data.breakdown.clinical_plausibility);

  // Render Issues Table
  const tbody = document.getElementById('issues-body');
  tbody.innerHTML = '';

  const severityMap = {
    low: { color: "#2e7d32", label: "Low" },     // Green
    medium: { color: "#ef6c00", label: "Medium" }, // Orange
    high: { color: "#c62828", label: "High" }    // Red
  };

  data.issues_detected.forEach(item => {
    const row = tbody.insertRow();
    const style = severityMap[item.severity] || severityMap.low;

    row.innerHTML = `
      <td><code>${item.field}</code></td>
      <td>${item.issue}</td>
      <td>
        <span style="background-color: ${style.color}; color: white; padding: 2px 10px; border-radius: 10px; font-size: 0.75rem; font-weight: bold; text-transform: uppercase;">
          ${style.label}
        </span>
      </td>
      `;
  });
}

async function submitValidation() {
  const getTags = (id) => Array.from(document.querySelectorAll(`#${id} .tag`)).map(t => t.innerText.replace('x', '').trim());

  const data = {
    name: document.getElementById("validate-patient-name").value,
    dob: document.getElementById("validate-patient-dob").value,
    gender: document.getElementById("validate-patient-gender").value,
    conditions: getTags("validate-condition-tags"),
    allergies: getTags("validate-allergy-tags"),
    medications: getTags("validate-med-tags"),
    vitals: getTags("validate-vitals-tags"),
    last_updated: document.getElementById("validate-last-updated").value
  };

  try {
    response = await fetch('/api/validate/data_quality', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(data)
    });

    if (!response.ok) {
      const errorData = await response.json();
      console.error("Server Error:", errorData);
      submitValidation();
    }

    const jsonData = await response.json();
    displayValidationResults(jsonData);
  }
  catch (error) {
    console.error("Fetch error: ", error);
  }
}


function showPage(pageId) {
  // Hide all pages
  document.querySelectorAll('.page-content').forEach(page => {
    page.classList.remove('active');
  });
  // Show the target page
  document.getElementById(pageId).classList.add('active');
}

window.addEventListener('DOMContentLoaded', () => {
  const isAuthorized = sessionStorage.getItem('loginSuccess');

  if (isAuthorized !== 'true') {
    // User didn't come from the login page, get them out
    window.location.href = '/api/login'; 
  } else {
    console.log("Access granted.");
  }
});
